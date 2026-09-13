#include "telemetry/telemetry_session.h"

#include <limits>

namespace
{

constexpr telemetry_record_key ZERO_RECORD_KEY = {};
constexpr telemetry_session_ref ZERO_SESSION_REF = {};
constexpr telemetry_connection_id ZERO_CONNECTION_ID = {};

bool producer_equal(const telemetry_producer_id &left, const telemetry_producer_id &right) noexcept
{
	return left.boot_id == right.boot_id && left.process_id == right.process_id;
}

bool session_id_equal(const telemetry_session_id &left, const telemetry_session_id &right) noexcept
{
	return producer_equal(left.producer, right.producer) &&
	       left.session_seq == right.session_seq;
}

bool session_ref_equal(const telemetry_session_ref &left,
		       const telemetry_session_ref &right) noexcept
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

bool counters_equal(const telemetry_cumulative_counters &left,
		    const telemetry_cumulative_counters &right) noexcept
{
	return left.connected_usec == right.connected_usec &&
	       left.active_usec == right.active_usec && left.idle_usec == right.idle_usec &&
	       left.unknown_usec == right.unknown_usec &&
	       left.resident_usec == right.resident_usec &&
	       left.linkdead_usec == right.linkdead_usec;
}

bool enter_equal(const telemetry_session_enter &left, const telemetry_session_enter &right) noexcept
{
	return session_ref_equal(left.session, right.session) &&
	       connection_equal(left.connection, right.connection) &&
	       left.at_monotonic_usec == right.at_monotonic_usec &&
	       left.at_utc_usec == right.at_utc_usec &&
	       dimensions_equal(left.dimensions, right.dimensions) &&
	       left.config_id == right.config_id &&
	       left.classifier_version == right.classifier_version &&
	       left.policy_version == right.policy_version &&
	       left.quality_flags == right.quality_flags;
}

bool handoff_equal(const telemetry_session_handoff &left,
		   const telemetry_session_handoff &right) noexcept
{
	return session_ref_equal(left.session, right.session) &&
	       producer_equal(left.previous_producer, right.previous_producer) &&
	       left.last_checkpoint_revision == right.last_checkpoint_revision &&
	       counters_equal(left.cumulative, right.cumulative) &&
	       left.quality_flags == right.quality_flags;
}

bool transition_equal(const telemetry_connection_transition &left,
		      const telemetry_connection_transition &right) noexcept
{
	return session_ref_equal(left.session, right.session) &&
	       connection_equal(left.connection, right.connection) &&
	       left.at_monotonic_usec == right.at_monotonic_usec &&
	       left.at_utc_usec == right.at_utc_usec && left.kind == right.kind &&
	       left.reserved[0] == right.reserved[0] && left.reserved[1] == right.reserved[1] &&
	       left.reserved[2] == right.reserved[2] && left.quality_flags == right.quality_flags;
}

bool counter_equal(const telemetry_counter_update &left,
		   const telemetry_counter_update &right) noexcept
{
	return session_ref_equal(left.session, right.session) &&
	       connection_equal(left.connection, right.connection) &&
	       left.at_monotonic_usec == right.at_monotonic_usec &&
	       left.connected_delta_usec == right.connected_delta_usec &&
	       left.active_delta_usec == right.active_delta_usec &&
	       left.idle_delta_usec == right.idle_delta_usec &&
	       left.unknown_delta_usec == right.unknown_delta_usec &&
	       left.resident_delta_usec == right.resident_delta_usec &&
	       left.linkdead_delta_usec == right.linkdead_delta_usec &&
	       left.quality_flags == right.quality_flags;
}

bool exit_equal(const telemetry_session_exit &left, const telemetry_session_exit &right) noexcept
{
	return session_ref_equal(left.session, right.session) &&
	       connection_equal(left.connection, right.connection) &&
	       left.at_monotonic_usec == right.at_monotonic_usec &&
	       left.at_utc_usec == right.at_utc_usec && left.reason == right.reason &&
	       left.reserved[0] == right.reserved[0] && left.reserved[1] == right.reserved[1] &&
	       left.reserved[2] == right.reserved[2] && left.quality_flags == right.quality_flags;
}

bool cumulative_add(telemetry_cumulative_counters &total,
		    const telemetry_cumulative_counters &delta) noexcept
{
	const auto add =
		[](telemetry_duration_usec &target, telemetry_duration_usec amount) noexcept
	{
		if (amount > std::numeric_limits<telemetry_duration_usec>::max() - target)
			return false;
		target += amount;
		return true;
	};
	auto candidate = total;
	if (!add(candidate.connected_usec, delta.connected_usec) ||
	    !add(candidate.active_usec, delta.active_usec) ||
	    !add(candidate.idle_usec, delta.idle_usec) ||
	    !add(candidate.unknown_usec, delta.unknown_usec) ||
	    !add(candidate.resident_usec, delta.resident_usec) ||
	    !add(candidate.linkdead_usec, delta.linkdead_usec) ||
	    !telemetry_cumulative_counters_are_valid(candidate))
		return false;
	total = candidate;
	return true;
}

void saturating_increment(std::uint64_t &target) noexcept
{
	if (target != std::numeric_limits<std::uint64_t>::max())
		++target;
}

telemetry_session_state_result result_with(telemetry_session_state_outcome outcome) noexcept
{
	telemetry_session_state_result result{};
	result.outcome = outcome;
	return result;
}

void result_note_key(telemetry_session_state_result &result, const telemetry_record_key &key,
		     bool accepted) noexcept
{
	if (result.records_attempted != std::numeric_limits<std::uint16_t>::max())
		++result.records_attempted;
	if (!telemetry_record_key_is_valid(result.first_record))
		result.first_record = key;
	result.last_record = key;
	if (accepted)
	{
		if (result.records_accepted != std::numeric_limits<std::uint16_t>::max())
			++result.records_accepted;
	}
	else if (result.records_dropped != std::numeric_limits<std::uint16_t>::max())
		++result.records_dropped;
}

void result_note_unallocated_drop(telemetry_session_state_result &result) noexcept
{
	if (result.records_dropped != std::numeric_limits<std::uint16_t>::max())
		++result.records_dropped;
}

void result_merge(telemetry_session_state_result &target,
		  const telemetry_session_state_result &source) noexcept
{
	if (source.records_attempted >
	    std::numeric_limits<std::uint16_t>::max() - target.records_attempted)
		target.records_attempted = std::numeric_limits<std::uint16_t>::max();
	else
		target.records_attempted += source.records_attempted;
	if (source.records_accepted >
	    std::numeric_limits<std::uint16_t>::max() - target.records_accepted)
		target.records_accepted = std::numeric_limits<std::uint16_t>::max();
	else
		target.records_accepted += source.records_accepted;
	if (source.records_dropped >
	    std::numeric_limits<std::uint16_t>::max() - target.records_dropped)
		target.records_dropped = std::numeric_limits<std::uint16_t>::max();
	else
		target.records_dropped += source.records_dropped;
	if (source.slots_retired > std::numeric_limits<std::uint16_t>::max() - target.slots_retired)
		target.slots_retired = std::numeric_limits<std::uint16_t>::max();
	else
		target.slots_retired += source.slots_retired;
	if (!telemetry_record_key_is_valid(target.first_record))
		target.first_record = source.first_record;
	if (telemetry_record_key_is_valid(source.last_record))
		target.last_record = source.last_record;
	if (source.revision > target.revision)
		target.revision = source.revision;
	target.quality_flags |= source.quality_flags;
	target.cumulative = source.cumulative;
}

void result_from_slot(telemetry_session_state_result &result,
		      const telemetry_session_slot &slot) noexcept
{
	result.cumulative = slot.cumulative;
	result.revision = slot.last_allocated_revision;
	result.quality_flags |= slot.quality_flags;
}

bool state_ready(const telemetry_session_state *state) noexcept
{
	return state != nullptr && state->initialized != 0U && state->max_slots > 0U &&
	       state->max_slots <= TELEMETRY_SESSION_STATE_MAX_SLOTS;
}

int find_slot(const telemetry_session_state *state, const telemetry_session_ref &session) noexcept
{
	for (std::size_t index = 0; index < state->max_slots; ++index)
	{
		const telemetry_session_slot &slot = state->slots[index];
		if (slot.lifecycle == telemetry_session_slot_lifecycle::empty)
			continue;
		if (!session_id_equal(slot.session.id, session.id))
			continue;
		return session_ref_equal(slot.session, session) ? static_cast<int>(index) : -2;
	}
	return -1;
}

void clear_slot(telemetry_session_state *state, std::size_t index) noexcept
{
	telemetry_session_slot &slot = state->slots[index];
	if (slot.lifecycle == telemetry_session_slot_lifecycle::empty)
		return;
	if (slot.lifecycle == telemetry_session_slot_lifecycle::resident &&
	    state->resident_slots != 0U)
		--state->resident_slots;
	if (slot.lifecycle == telemetry_session_slot_lifecycle::closed && state->closed_slots != 0U)
		--state->closed_slots;
	if (state->used_slots != 0U)
		--state->used_slots;
	slot = {};
}

std::size_t find_empty_slot(const telemetry_session_state *state) noexcept
{
	for (std::size_t index = 0; index < state->max_slots; ++index)
		if (state->slots[index].lifecycle == telemetry_session_slot_lifecycle::empty)
			return index;
	return state->max_slots;
}

std::size_t find_closed_slot(const telemetry_session_state *state) noexcept
{
	for (std::size_t index = 0; index < state->max_slots; ++index)
		if (state->slots[index].lifecycle == telemetry_session_slot_lifecycle::closed)
			return index;
	return state->max_slots;
}

std::size_t find_expired_detached_slot(const telemetry_session_state *state,
				       telemetry_monotonic_usec now) noexcept
{
	std::size_t best = state->max_slots;
	telemetry_monotonic_usec oldest = std::numeric_limits<telemetry_monotonic_usec>::max();
	for (std::size_t index = 0; index < state->max_slots; ++index)
	{
		const telemetry_session_slot &slot = state->slots[index];
		if (slot.lifecycle != telemetry_session_slot_lifecycle::resident ||
		    slot.connected != 0U || now < slot.detached_since_monotonic_usec ||
		    now - slot.detached_since_monotonic_usec < state->detached_retire_after_usec)
			continue;
		if (best == state->max_slots || slot.detached_since_monotonic_usec < oldest)
		{
			best = index;
			oldest = slot.detached_since_monotonic_usec;
		}
	}
	return best;
}

void note_pending_control_drop(telemetry_session_state *state, const telemetry_session_ref *session,
			       const telemetry_connection_id *connection,
			       telemetry_quality_mask quality) noexcept
{
	const bool first_drop = state->pending_control_drops == 0U;
	const bool scoped = session != nullptr && telemetry_session_ref_is_valid(*session);
	const telemetry_connection_id scope_connection =
		connection != nullptr && telemetry_connection_reference_is_valid(*connection) ?
			*connection :
			ZERO_CONNECTION_ID;
	if (first_drop)
	{
		state->pending_gap_has_scope = scoped ? 1U : 0U;
		state->pending_gap_session = scoped ? *session : ZERO_SESSION_REF;
		state->pending_gap_connection = scoped ? scope_connection : ZERO_CONNECTION_ID;
	}
	else if (state->pending_gap_has_scope != 0U &&
		 (!scoped || !session_ref_equal(state->pending_gap_session, *session) ||
		  !connection_equal(state->pending_gap_connection, scope_connection)))
	{
		// Mixed losses cannot all be attributed to the last affected session.
		state->pending_gap_has_scope = 0U;
		state->pending_gap_session = ZERO_SESSION_REF;
		state->pending_gap_connection = ZERO_CONNECTION_ID;
	}
	saturating_increment(state->pending_control_drops);
	saturating_increment(state->dropped_control_total);
	state->pending_gap_quality_flags |= quality | TELEMETRY_QUALITY_QUEUE_DROP;
	if (scoped)
	{
		const int index = find_slot(state, *session);
		if (index >= 0)
			state->slots[index].quality_flags |= quality | TELEMETRY_QUALITY_QUEUE_DROP;
	}
}

bool allocate_key(telemetry_session_state *state, telemetry_record_kind kind,
		  telemetry_record &record, telemetry_session_state_result &result,
		  const telemetry_session_ref *session,
		  const telemetry_connection_id *connection) noexcept
{
	telemetry_record_key key{};
	if (state->key_allocator.next == nullptr ||
	    !state->key_allocator.next(state->key_allocator.context, kind, &key) ||
	    !telemetry_record_key_is_valid(key) || !producer_equal(key.producer, state->producer))
	{
		note_pending_control_drop(state, session, connection,
					  TELEMETRY_QUALITY_SEQUENCE_GAP);
		result_note_unallocated_drop(result);
		return false;
	}
	record.header.key = key;
	return true;
}

bool submit_record(telemetry_session_state *state, telemetry_record &record,
		   telemetry_session_state_result &result, const telemetry_session_ref *session,
		   const telemetry_connection_id *connection) noexcept
{
	const bool accepted = state->sink.emit != nullptr &&
			      state->sink.emit(state->sink.context, &record);
	result_note_key(result, record.header.key, accepted);
	if (accepted)
		saturating_increment(state->accepted_control_total);
	else
		note_pending_control_drop(state, session, connection, TELEMETRY_QUALITY_QUEUE_DROP);
	return accepted;
}

bool emit_record(telemetry_session_state *state, telemetry_record &record,
		 telemetry_session_state_result &result, const telemetry_session_ref *session,
		 const telemetry_connection_id *connection) noexcept
{
	if (!allocate_key(state, record.header.kind, record, result, session, connection))
		return false;
	return submit_record(state, record, result, session, connection);
}

bool emit_pending_gap(telemetry_session_state *state, telemetry_session_state_result &result,
		      telemetry_utc_usec occurrence_utc_usec,
		      const telemetry_session_ref *fallback_session,
		      const telemetry_connection_id *fallback_connection) noexcept
{
	if (state->pending_control_drops == 0U)
		return true;
	telemetry_record record{};
	record.header.schema_version = TELEMETRY_SCHEMA_VERSION;
	record.header.kind = telemetry_record_kind::coverage_gap;
	record.header.occurrence_utc_usec = occurrence_utc_usec;
	telemetry_coverage_gap_payload &gap = record.payload.gap;
	(void)fallback_session;
	(void)fallback_connection;
	gap.session = state->pending_gap_has_scope != 0U ? state->pending_gap_session :
							   ZERO_SESSION_REF;
	gap.connection = state->pending_gap_has_scope != 0U ? state->pending_gap_connection :
							      ZERO_CONNECTION_ID;
	gap.reason = telemetry_gap_reason::control_queue_drop;
	gap.start_monotonic_usec = 0U;
	gap.end_monotonic_usec = 0U;
	gap.start_utc_usec = occurrence_utc_usec;
	gap.end_utc_usec = occurrence_utc_usec;
	gap.first_missing_record_seq = 0U;
	gap.last_missing_record_seq = 0U;
	gap.duration_usec = 0U;
	gap.dropped_records = state->pending_control_drops;
	gap.quality_flags = state->pending_gap_quality_flags | TELEMETRY_QUALITY_QUEUE_DROP |
			    TELEMETRY_QUALITY_SEQUENCE_GAP;
	const std::uint64_t prior_drops = state->pending_control_drops;
	const bool accepted =
		emit_record(state, record, result,
			    telemetry_session_ref_is_valid(gap.session) ? &gap.session : nullptr,
			    &gap.connection);
	if (accepted)
	{
		state->pending_control_drops = 0U;
		state->pending_gap_has_scope = 0U;
		state->pending_gap_session = ZERO_SESSION_REF;
		state->pending_gap_connection = ZERO_CONNECTION_ID;
		state->pending_gap_quality_flags = TELEMETRY_QUALITY_NONE;
	}
	else if (state->pending_control_drops < prior_drops)
		state->pending_control_drops = prior_drops;
	return accepted;
}

bool emit_unclosed_gap(telemetry_session_state *state, telemetry_session_state_result &result,
		       telemetry_utc_usec occurrence_utc_usec, const telemetry_session_ref &session,
		       const telemetry_connection_id &connection) noexcept
{
	(void)emit_pending_gap(state, result, occurrence_utc_usec,
			       telemetry_session_ref_is_valid(session) ? &session : nullptr,
			       &connection);
	telemetry_record record{};
	record.header.schema_version = TELEMETRY_SCHEMA_VERSION;
	record.header.kind = telemetry_record_kind::coverage_gap;
	record.header.occurrence_utc_usec = occurrence_utc_usec;
	telemetry_coverage_gap_payload &gap = record.payload.gap;
	gap.session = session;
	gap.connection = connection;
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
	return emit_record(state, record, result,
			   telemetry_session_ref_is_valid(session) ? &gap.session : nullptr,
			   &gap.connection);
}

telemetry_quality_mask utc_quality(const telemetry_session_slot &slot,
				   telemetry_monotonic_usec monotonic,
				   telemetry_utc_usec utc) noexcept
{
	if (utc == TELEMETRY_UTC_UNKNOWN || slot.last_observed_utc_usec == TELEMETRY_UTC_UNKNOWN)
		return TELEMETRY_QUALITY_NONE;
	if (utc < slot.last_observed_utc_usec || monotonic < slot.last_utc_monotonic_usec)
		return TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
	// Unsigned subtraction represents even a signed UTC range crossing zero.
	const auto utc_elapsed = static_cast<std::uint64_t>(utc) -
				 static_cast<std::uint64_t>(slot.last_observed_utc_usec);
	const auto monotonic_elapsed = monotonic - slot.last_utc_monotonic_usec;
	const auto drift = utc_elapsed > monotonic_elapsed ? utc_elapsed - monotonic_elapsed :
							     monotonic_elapsed - utc_elapsed;
	// Paired clock reads may have sampling skew. Callers can mark finer jumps
	// explicitly; this conservative control-record detector ignores <= 1 second.
	return drift > UINT64_C(1000000) ? TELEMETRY_QUALITY_CLOCK_DISCONTINUITY :
					   TELEMETRY_QUALITY_NONE;
}

void remember_utc(telemetry_session_slot &slot, telemetry_monotonic_usec monotonic,
		  telemetry_utc_usec utc) noexcept
{
	slot.last_observed_utc_usec = utc;
	slot.last_utc_monotonic_usec = monotonic;
}

bool account_elapsed_to(telemetry_session_slot &slot, telemetry_monotonic_usec at_monotonic_usec,
			telemetry_utc_usec at_utc_usec, telemetry_quality_mask &quality) noexcept
{
	if (at_monotonic_usec < slot.last_observed_monotonic_usec)
		return false;
	const telemetry_duration_usec elapsed =
		at_monotonic_usec - slot.last_observed_monotonic_usec;
	telemetry_cumulative_counters delta{};
	// #266 owns classification. Without a supplied delta, never extrapolate
	// the previous active/idle category across unobserved connected time.
	if (slot.connected != 0U)
		delta = { elapsed, 0U, 0U, elapsed, elapsed, 0U };
	else
		delta = { 0U, 0U, 0U, 0U, elapsed, elapsed };
	if (!cumulative_add(slot.cumulative, delta) ||
	    !telemetry_cumulative_counters_are_valid(slot.cumulative))
		return false;
	quality |= utc_quality(slot, at_monotonic_usec, at_utc_usec);
	slot.last_observed_monotonic_usec = at_monotonic_usec;
	remember_utc(slot, at_monotonic_usec, at_utc_usec);
	return true;
}

void set_result_failure(telemetry_session_state_result &result,
			telemetry_session_state_outcome outcome) noexcept
{
	result.outcome = outcome;
}

void initialize_slot(telemetry_session_slot &slot, const telemetry_session_enter &enter,
		     const telemetry_cumulative_counters &cumulative,
		     telemetry_checkpoint_revision last_revision,
		     telemetry_quality_mask quality) noexcept
{
	slot = {};
	slot.lifecycle = telemetry_session_slot_lifecycle::resident;
	slot.mode = telemetry_session_accounting_mode::unknown;
	slot.connected = 1U;
	slot.session = enter.session;
	slot.connection = enter.connection;
	slot.entry_connection = enter.connection;
	slot.dimensions = enter.dimensions;
	slot.config_id = enter.config_id;
	slot.classifier_version = enter.classifier_version;
	slot.policy_version = enter.policy_version;
	slot.quality_flags = quality;
	slot.cumulative = cumulative;
	slot.entry_monotonic_usec = enter.at_monotonic_usec;
	slot.entry_utc_usec = enter.at_utc_usec;
	slot.last_observed_monotonic_usec = enter.at_monotonic_usec;
	slot.last_observed_utc_usec = enter.at_utc_usec;
	slot.last_utc_monotonic_usec = enter.at_monotonic_usec;
	slot.last_allocated_revision = last_revision;
	slot.entry_snapshot = enter;
}

void occupy_slot(telemetry_session_state *state, std::size_t index) noexcept
{
	telemetry_session_slot &slot = state->slots[index];
	if (slot.lifecycle == telemetry_session_slot_lifecycle::resident)
	{
		++state->used_slots;
		++state->resident_slots;
		state->last_connection_sequence = slot.connection.connection_seq;
		if (producer_equal(slot.session.id.producer, state->producer))
			state->last_local_session_sequence = slot.session.id.session_seq;
	}
}

bool prepare_new_slot(telemetry_session_state *state, telemetry_monotonic_usec now_monotonic_usec,
		      telemetry_utc_usec now_utc_usec, telemetry_session_state_result &result,
		      std::size_t &index) noexcept
{
	index = find_empty_slot(state);
	if (index < state->max_slots)
		return true;
	index = find_closed_slot(state);
	if (index < state->max_slots)
	{
		clear_slot(state, index);
		return true;
	}
	index = find_expired_detached_slot(state, now_monotonic_usec);
	if (index >= state->max_slots)
		return false;
	telemetry_session_slot &old = state->slots[index];
	const telemetry_session_ref old_session = old.session;
	const telemetry_connection_id old_connection = ZERO_CONNECTION_ID;
	(void)emit_unclosed_gap(state, result, now_utc_usec, old_session, old_connection);
	clear_slot(state, index);
	if (result.slots_retired != std::numeric_limits<std::uint16_t>::max())
		++result.slots_retired;
	saturating_increment(state->retired_session_total);
	saturating_increment(state->unclosed_tail_total);
	return true;
}

bool session_is_new_for_process(const telemetry_session_state *state,
				const telemetry_session_enter &enter) noexcept
{
	return producer_equal(enter.session.id.producer, state->producer) &&
	       producer_equal(enter.connection.producer, state->producer);
}

bool handoff_is_zero(const telemetry_session_handoff &handoff) noexcept
{
	return telemetry_session_ref_is_zero(handoff.session) &&
	       telemetry_producer_id_is_zero(handoff.previous_producer) &&
	       handoff.last_checkpoint_revision == 0U &&
	       counters_equal(handoff.cumulative, telemetry_cumulative_counters{}) &&
	       handoff.quality_flags == TELEMETRY_QUALITY_NONE;
}

telemetry_session_state_result
emit_lifecycle(telemetry_session_state *state, telemetry_session_slot &slot,
	       telemetry_lifecycle_kind lifecycle, telemetry_connection_id connection,
	       telemetry_session_end_reason reason, telemetry_monotonic_usec at_monotonic_usec,
	       telemetry_utc_usec at_utc_usec, telemetry_quality_mask quality) noexcept
{
	telemetry_session_state_result result =
		result_with(telemetry_session_state_outcome::accepted);
	(void)emit_pending_gap(state, result, at_utc_usec, &slot.session, &connection);
	telemetry_record record{};
	record.header.schema_version = TELEMETRY_SCHEMA_VERSION;
	record.header.kind = telemetry_record_kind::session_lifecycle;
	record.header.occurrence_utc_usec = at_utc_usec;
	telemetry_session_lifecycle_payload &payload = record.payload.lifecycle;
	payload.session = slot.session;
	payload.connection = connection;
	payload.lifecycle = lifecycle;
	payload.end_reason = reason;
	payload.at_monotonic_usec = at_monotonic_usec;
	payload.at_utc_usec = at_utc_usec;
	payload.dimensions = slot.dimensions;
	payload.config_id = slot.config_id;
	payload.classifier_version = slot.classifier_version;
	payload.policy_version = slot.policy_version;
	payload.quality_flags = quality | slot.quality_flags;
	const std::uint16_t attempts_before = result.records_attempted;
	if (!emit_record(state, record, result, &slot.session, &connection))
		set_result_failure(result,
				   result.records_attempted == attempts_before ?
					   telemetry_session_state_outcome::allocator_exhausted :
					   telemetry_session_state_outcome::sink_rejected);
	else
		result.outcome = telemetry_session_state_outcome::accepted;
	result_from_slot(result, slot);
	return result;
}

telemetry_session_state_result checkpoint_at_slot(telemetry_session_state *state,
						  telemetry_session_slot &slot,
						  telemetry_monotonic_usec at_monotonic_usec,
						  telemetry_utc_usec at_utc_usec) noexcept
{
	telemetry_session_state_result result =
		result_with(telemetry_session_state_outcome::accepted);
	const telemetry_connection_id connection = slot.connected != 0U ? slot.connection :
									  ZERO_CONNECTION_ID;
	const telemetry_quality_mask quality = slot.quality_flags |
					       utc_quality(slot, at_monotonic_usec, at_utc_usec);
	if (slot.has_last_checkpoint != 0U &&
	    slot.last_checkpoint_monotonic_usec == at_monotonic_usec &&
	    slot.last_checkpoint_utc_usec == at_utc_usec &&
	    connection_equal(slot.last_checkpoint_connection, connection) &&
	    counters_equal(slot.last_checkpoint_cumulative, slot.cumulative) &&
	    slot.last_checkpoint_quality_flags == quality)
	{
		result.outcome = telemetry_session_state_outcome::idempotent;
		result_from_slot(result, slot);
		return result;
	}
	if (at_monotonic_usec < slot.last_observed_monotonic_usec)
	{
		set_result_failure(result, telemetry_session_state_outcome::invalid);
		result_from_slot(result, slot);
		return result;
	}
	if (slot.last_allocated_revision ==
	    std::numeric_limits<telemetry_checkpoint_revision>::max())
	{
		set_result_failure(result, telemetry_session_state_outcome::revision_exhausted);
		saturating_increment(state->revision_exhaustion_total);
		result_from_slot(result, slot);
		return result;
	}
	telemetry_quality_mask elapsed_quality = quality;
	if (!account_elapsed_to(slot, at_monotonic_usec, at_utc_usec, elapsed_quality))
	{
		result.outcome = telemetry_session_state_outcome::invalid;
		result_from_slot(result, slot);
		return result;
	}
	slot.quality_flags |= elapsed_quality;
	(void)emit_pending_gap(state, result, at_utc_usec, &slot.session, &connection);
	telemetry_record record{};
	record.header.schema_version = TELEMETRY_SCHEMA_VERSION;
	record.header.kind = telemetry_record_kind::session_checkpoint;
	record.header.occurrence_utc_usec = at_utc_usec;
	telemetry_session_checkpoint_payload &payload = record.payload.checkpoint;
	payload.session = slot.session;
	payload.connection = connection;
	payload.revision = slot.last_allocated_revision + 1U;
	payload.at_monotonic_usec = at_monotonic_usec;
	payload.at_utc_usec = at_utc_usec;
	payload.cumulative = slot.cumulative;
	payload.config_id = slot.config_id;
	payload.quality_flags = quality | slot.quality_flags;
	if (!allocate_key(state, record.header.kind, record, result, &slot.session, &connection))
	{
		set_result_failure(result, telemetry_session_state_outcome::allocator_exhausted);
		result_from_slot(result, slot);
		return result;
	}
	slot.last_allocated_revision = payload.revision;
	slot.has_last_checkpoint = 1U;
	slot.last_checkpoint_connection = connection;
	slot.last_checkpoint_cumulative = slot.cumulative;
	slot.last_checkpoint_monotonic_usec = at_monotonic_usec;
	slot.last_checkpoint_utc_usec = at_utc_usec;
	slot.last_checkpoint_quality_flags = payload.quality_flags;
	slot.quality_flags |= utc_quality(slot, at_monotonic_usec, at_utc_usec);
	const bool accepted = submit_record(state, record, result, &slot.session, &connection);
	result.outcome = accepted ? telemetry_session_state_outcome::accepted :
				    telemetry_session_state_outcome::sink_rejected;
	result_from_slot(result, slot);
	return result;
}

telemetry_session_state_result retire_one_at(telemetry_session_state *state, std::size_t index,
					     telemetry_utc_usec occurrence_utc_usec) noexcept
{
	telemetry_session_state_result result =
		result_with(telemetry_session_state_outcome::retired);
	telemetry_session_slot &slot = state->slots[index];
	const telemetry_session_ref session = slot.session;
	(void)emit_unclosed_gap(state, result, occurrence_utc_usec, session, ZERO_CONNECTION_ID);
	clear_slot(state, index);
	if (result.slots_retired != std::numeric_limits<std::uint16_t>::max())
		++result.slots_retired;
	saturating_increment(state->retired_session_total);
	saturating_increment(state->unclosed_tail_total);
	return result;
}

} // namespace

telemetry_session_state_outcome
telemetry_session_state_init(telemetry_session_state *state,
			     const telemetry_session_state_config *config) noexcept
{
	if (state == nullptr || config == nullptr || config->reserved != 0U ||
	    config->max_slots == 0U || config->max_slots > TELEMETRY_SESSION_STATE_MAX_SLOTS ||
	    config->detached_retire_after_usec == 0U ||
	    config->detached_retire_after_usec > TELEMETRY_SESSION_RETIRE_AFTER_MAX_USEC ||
	    !telemetry_producer_id_is_valid(config->producer) || config->clock.now == nullptr ||
	    config->sink.emit == nullptr || config->key_allocator.next == nullptr)
		return telemetry_session_state_outcome::invalid;
	*state = {};
	state->max_slots = config->max_slots;
	state->detached_retire_after_usec = config->detached_retire_after_usec;
	state->producer = config->producer;
	state->clock = config->clock;
	state->sink = config->sink;
	state->key_allocator = config->key_allocator;
	state->initialized = 1U;
	return telemetry_session_state_outcome::accepted;
}

void telemetry_session_state_reset(telemetry_session_state *state) noexcept
{
	if (state != nullptr)
		*state = {};
}

telemetry_session_state_result telemetry_session_state_enter(telemetry_session_state *state,
							     telemetry_session_enter enter) noexcept
{
	telemetry_session_state_result result =
		result_with(telemetry_session_state_outcome::invalid);
	if (!state_ready(state) || !telemetry_session_enter_is_valid(enter) ||
	    !session_is_new_for_process(state, enter))
		return result;
	const int existing = find_slot(state, enter.session);
	if (existing == -2)
		return result;
	if (existing >= 0)
	{
		telemetry_session_slot &slot = state->slots[static_cast<std::size_t>(existing)];
		if (slot.has_last_exit != 0U && enter_equal(slot.entry_snapshot, enter))
		{
			result.outcome = telemetry_session_state_outcome::idempotent;
			result_from_slot(result, slot);
			return result;
		}
		if (slot.lifecycle == telemetry_session_slot_lifecycle::resident &&
		    enter_equal(slot.entry_snapshot, enter))
		{
			result.outcome = telemetry_session_state_outcome::idempotent;
			result_from_slot(result, slot);
			return result;
		}
		return result;
	}
	if (enter.connection.connection_seq <= state->last_connection_sequence ||
	    enter.session.id.session_seq <= state->last_local_session_sequence)
		return result;
	std::size_t index = state->max_slots;
	if (!prepare_new_slot(state, enter.at_monotonic_usec, enter.at_utc_usec, result, index))
	{
		result.outcome = telemetry_session_state_outcome::capacity_full;
		return result;
	}
	telemetry_session_slot &slot = state->slots[index];
	initialize_slot(slot, enter, telemetry_cumulative_counters{}, 0U, enter.quality_flags);
	occupy_slot(state, index);
	telemetry_session_state_result emitted =
		emit_lifecycle(state, slot, telemetry_lifecycle_kind::session_entered,
			       slot.connection, telemetry_session_end_reason::unknown,
			       enter.at_monotonic_usec, enter.at_utc_usec, enter.quality_flags);
	result_merge(result, emitted);
	result.outcome = emitted.outcome == telemetry_session_state_outcome::accepted ?
				 telemetry_session_state_outcome::accepted :
				 emitted.outcome;
	result_from_slot(result, slot);
	return result;
}

telemetry_session_state_result
telemetry_session_state_transition(telemetry_session_state *state,
				   telemetry_connection_transition transition) noexcept
{
	telemetry_session_state_result result =
		result_with(telemetry_session_state_outcome::invalid);
	if (!state_ready(state) || !telemetry_connection_transition_is_valid(transition) ||
	    transition.kind == telemetry_connection_transition_kind::copyover_resumed ||
	    !producer_equal(transition.connection.producer, state->producer))
		return result;
	const int found = find_slot(state, transition.session);
	if (found < 0)
		return found == -1 ? result_with(telemetry_session_state_outcome::not_found) :
				     result;
	telemetry_session_slot &slot = state->slots[static_cast<std::size_t>(found)];
	if (slot.lifecycle == telemetry_session_slot_lifecycle::closed)
	{
		if (slot.has_last_transition != 0U &&
		    transition_equal(slot.last_transition_snapshot, transition))
			result.outcome = telemetry_session_state_outcome::idempotent;
		return result;
	}
	if (slot.has_last_transition != 0U &&
	    transition_equal(slot.last_transition_snapshot, transition))
	{
		result.outcome = telemetry_session_state_outcome::idempotent;
		result_from_slot(result, slot);
		return result;
	}
	if (transition.kind == telemetry_connection_transition_kind::attached)
	{
		if (slot.connected != 0U ||
		    transition.connection.connection_seq <= state->last_connection_sequence)
			return result;
		if (transition.at_monotonic_usec < slot.last_observed_monotonic_usec)
			return result;
		telemetry_quality_mask quality = transition.quality_flags;
		if (!account_elapsed_to(slot, transition.at_monotonic_usec, transition.at_utc_usec,
					quality))
			return result;
		state->last_connection_sequence = transition.connection.connection_seq;
		slot.connection = transition.connection;
		slot.connected = 1U;
		slot.mode = telemetry_session_accounting_mode::unknown;
		slot.detached_since_monotonic_usec = 0U;
		slot.quality_flags |= quality;
		slot.last_transition_snapshot = transition;
		slot.has_last_transition = 1U;
		telemetry_session_state_result emitted = emit_lifecycle(
			state, slot, telemetry_lifecycle_kind::connection_attached, slot.connection,
			telemetry_session_end_reason::unknown, transition.at_monotonic_usec,
			transition.at_utc_usec, quality);
		result_merge(result, emitted);
		result.outcome = emitted.outcome;
		result_from_slot(result, slot);
		return result;
	}
	if (slot.connected == 0U || !connection_equal(slot.connection, transition.connection))
		return result;
	if (transition.at_monotonic_usec < slot.last_observed_monotonic_usec)
		return result;
	telemetry_quality_mask quality = transition.quality_flags;
	if (!account_elapsed_to(slot, transition.at_monotonic_usec, transition.at_utc_usec,
				quality))
		return result;
	const telemetry_connection_id closing_connection = slot.connection;
	slot.connection = ZERO_CONNECTION_ID;
	slot.connected = 0U;
	slot.mode = telemetry_session_accounting_mode::linkdead;
	slot.detached_since_monotonic_usec = transition.at_monotonic_usec;
	slot.quality_flags |= quality;
	slot.last_transition_snapshot = transition;
	slot.has_last_transition = 1U;
	telemetry_session_state_result emitted =
		emit_lifecycle(state, slot, telemetry_lifecycle_kind::connection_detached,
			       closing_connection, telemetry_session_end_reason::unknown,
			       transition.at_monotonic_usec, transition.at_utc_usec, quality);
	result_merge(result, emitted);
	result.outcome = emitted.outcome;
	result_from_slot(result, slot);
	return result;
}

telemetry_session_state_result
telemetry_session_state_update_counters(telemetry_session_state *state,
					telemetry_counter_update update) noexcept
{
	telemetry_session_state_result result =
		result_with(telemetry_session_state_outcome::invalid);
	if (!state_ready(state) || !telemetry_counter_update_is_valid(update))
		return result;
	const int found = find_slot(state, update.session);
	if (found < 0)
		return found == -1 ? result_with(telemetry_session_state_outcome::not_found) :
				     result;
	telemetry_session_slot &slot = state->slots[static_cast<std::size_t>(found)];
	if (slot.lifecycle == telemetry_session_slot_lifecycle::closed)
	{
		if (slot.has_last_counter != 0U &&
		    counter_equal(slot.last_counter_snapshot, update))
			result.outcome = telemetry_session_state_outcome::idempotent;
		return result;
	}
	const telemetry_connection_id expected = slot.connected != 0U ? slot.connection :
									ZERO_CONNECTION_ID;
	if (!connection_equal(expected, update.connection) ||
	    (slot.connected != 0U && update.connection.producer.process_id == 0U) ||
	    (slot.has_last_counter != 0U && counter_equal(slot.last_counter_snapshot, update)))
	{
		if (slot.has_last_counter != 0U &&
		    counter_equal(slot.last_counter_snapshot, update))
		{
			result.outcome = telemetry_session_state_outcome::idempotent;
			result_from_slot(result, slot);
		}
		return result;
	}
	if (update.at_monotonic_usec < slot.last_observed_monotonic_usec ||
	    update.resident_delta_usec !=
		    update.at_monotonic_usec - slot.last_observed_monotonic_usec)
		return result;
	const telemetry_cumulative_counters delta = {
		update.connected_delta_usec, update.active_delta_usec,	 update.idle_delta_usec,
		update.unknown_delta_usec,   update.resident_delta_usec, update.linkdead_delta_usec
	};
	if (!cumulative_add(slot.cumulative, delta) ||
	    !telemetry_cumulative_counters_are_valid(slot.cumulative))
		return result;
	slot.last_observed_monotonic_usec = update.at_monotonic_usec;
	slot.mode = slot.connected != 0U ? telemetry_session_accounting_mode::unknown :
					   telemetry_session_accounting_mode::linkdead;
	slot.quality_flags |= update.quality_flags;
	slot.last_counter_snapshot = update;
	slot.has_last_counter = 1U;
	result.outcome = telemetry_session_state_outcome::accepted;
	result_from_slot(result, slot);
	return result;
}

telemetry_session_state_result
telemetry_session_state_checkpoint(telemetry_session_state *state,
				   telemetry_session_ref session) noexcept
{
	telemetry_session_state_result result =
		result_with(telemetry_session_state_outcome::invalid);
	if (!state_ready(state))
		return result;
	telemetry_monotonic_usec now_monotonic_usec = 0U;
	telemetry_utc_usec now_utc_usec = TELEMETRY_UTC_UNKNOWN;
	if (state->clock.now == nullptr ||
	    !state->clock.now(state->clock.context, &now_monotonic_usec, &now_utc_usec))
	{
		result.outcome = telemetry_session_state_outcome::clock_unavailable;
		return result;
	}
	return telemetry_session_state_checkpoint_at(state, session, now_monotonic_usec,
						     now_utc_usec);
}

telemetry_session_state_result
telemetry_session_state_checkpoint_at(telemetry_session_state *state, telemetry_session_ref session,
				      telemetry_monotonic_usec at_monotonic_usec,
				      telemetry_utc_usec at_utc_usec) noexcept
{
	telemetry_session_state_result result =
		result_with(telemetry_session_state_outcome::invalid);
	if (!state_ready(state))
		return result;
	const int found = find_slot(state, session);
	if (found < 0)
		return found == -1 ? result_with(telemetry_session_state_outcome::not_found) :
				     result;
	telemetry_session_slot &slot = state->slots[static_cast<std::size_t>(found)];
	if (slot.lifecycle != telemetry_session_slot_lifecycle::resident)
		return result;
	return checkpoint_at_slot(state, slot, at_monotonic_usec, at_utc_usec);
}

telemetry_session_state_result telemetry_session_state_exit(telemetry_session_state *state,
							    telemetry_session_exit exit) noexcept
{
	telemetry_session_state_result result =
		result_with(telemetry_session_state_outcome::invalid);
	if (!state_ready(state) || !telemetry_session_exit_is_valid(exit))
		return result;
	const int found = find_slot(state, exit.session);
	if (found < 0)
		return found == -1 ? result_with(telemetry_session_state_outcome::not_found) :
				     result;
	telemetry_session_slot &slot = state->slots[static_cast<std::size_t>(found)];
	if (slot.lifecycle == telemetry_session_slot_lifecycle::closed)
	{
		if (slot.has_last_exit != 0U && exit_equal(slot.last_exit_snapshot, exit))
		{
			result.outcome = telemetry_session_state_outcome::idempotent;
			result_from_slot(result, slot);
		}
		return result;
	}
	const telemetry_connection_id expected = slot.connected != 0U ? slot.connection :
									ZERO_CONNECTION_ID;
	if (!connection_equal(expected, exit.connection) ||
	    exit.at_monotonic_usec < slot.last_observed_monotonic_usec)
		return result;
	telemetry_quality_mask quality = exit.quality_flags;
	if (!account_elapsed_to(slot, exit.at_monotonic_usec, exit.at_utc_usec, quality))
		return result;
	slot.quality_flags |= quality;
	telemetry_session_state_result checkpoint =
		checkpoint_at_slot(state, slot, exit.at_monotonic_usec, exit.at_utc_usec);
	result_merge(result, checkpoint);
	const telemetry_connection_id closing_connection =
		slot.connected != 0U ? slot.connection : ZERO_CONNECTION_ID;
	telemetry_session_state_result emitted = emit_lifecycle(
		state, slot, telemetry_lifecycle_kind::session_exited, closing_connection,
		exit.reason, exit.at_monotonic_usec, exit.at_utc_usec, quality);
	result_merge(result, emitted);
	const telemetry_session_state_outcome exit_outcome = emitted.outcome;
	slot.last_exit_snapshot = exit;
	slot.has_last_exit = 1U;
	if (slot.lifecycle == telemetry_session_slot_lifecycle::resident &&
	    state->resident_slots != 0U)
		--state->resident_slots;
	++state->closed_slots;
	slot.lifecycle = telemetry_session_slot_lifecycle::closed;
	slot.connected = 0U;
	slot.connection = ZERO_CONNECTION_ID;
	slot.mode = telemetry_session_accounting_mode::unknown;
	slot.detached_since_monotonic_usec = 0U;
	result.outcome = exit_outcome == telemetry_session_state_outcome::accepted ?
				 telemetry_session_state_outcome::accepted :
				 exit_outcome;
	result_from_slot(result, slot);
	return result;
}

telemetry_session_state_result
telemetry_session_state_handoff_copy(telemetry_session_state *state, telemetry_session_ref session,
				     telemetry_session_handoff *handoff) noexcept
{
	telemetry_session_state_result result =
		result_with(telemetry_session_state_outcome::invalid);
	if (handoff != nullptr)
		*handoff = {};
	if (!state_ready(state) || handoff == nullptr || !telemetry_session_ref_is_valid(session))
		return result;
	const int found = find_slot(state, session);
	if (found < 0)
		return found == -1 ? result_with(telemetry_session_state_outcome::not_found) :
				     result;
	telemetry_session_slot &slot = state->slots[static_cast<std::size_t>(found)];
	if (slot.lifecycle == telemetry_session_slot_lifecycle::closed)
		return result_with(telemetry_session_state_outcome::not_found);
	telemetry_monotonic_usec now_monotonic_usec = 0U;
	telemetry_utc_usec now_utc_usec = TELEMETRY_UTC_UNKNOWN;
	if (state->clock.now == nullptr ||
	    !state->clock.now(state->clock.context, &now_monotonic_usec, &now_utc_usec))
		return result_with(telemetry_session_state_outcome::clock_unavailable);
	telemetry_quality_mask quality = slot.quality_flags;
	if (!account_elapsed_to(slot, now_monotonic_usec, now_utc_usec, quality))
		return result;
	slot.quality_flags |= quality;
	// A successful handoff must not silently discard explicit pending loss.
	// This is one nonblocking RAM admission attempt, never a durability wait.
	if (!emit_pending_gap(state, result, now_utc_usec, &slot.session, &slot.connection))
	{
		result.outcome = telemetry_record_key_is_valid(result.last_record) ?
					 telemetry_session_state_outcome::sink_rejected :
					 telemetry_session_state_outcome::allocator_exhausted;
		result_from_slot(result, slot);
		return result;
	}
	handoff->session = slot.session;
	handoff->previous_producer = state->producer;
	handoff->last_checkpoint_revision = slot.last_allocated_revision;
	handoff->cumulative = slot.cumulative;
	handoff->quality_flags = slot.quality_flags | (state->pending_control_drops != 0U ?
							       TELEMETRY_QUALITY_QUEUE_DROP :
							       TELEMETRY_QUALITY_NONE);
	result.outcome = telemetry_session_state_outcome::accepted;
	result_from_slot(result, slot);
	return result;
}

telemetry_session_state_result
telemetry_session_state_resume(telemetry_session_state *state,
			       telemetry_session_resume resume) noexcept
{
	telemetry_session_state_result result =
		result_with(telemetry_session_state_outcome::invalid);
	if (!state_ready(state) || !telemetry_session_enter_is_valid(resume.entry))
		return result;
	if (!producer_equal(resume.entry.connection.producer, state->producer))
		return result;
	const bool absent = handoff_is_zero(resume.handoff);
	if (!absent && (!telemetry_session_resume_is_valid(resume) ||
			producer_equal(resume.entry.session.id.producer, state->producer)))
		return result;
	if (absent && !session_is_new_for_process(state, resume.entry))
		return result;
	const int existing = find_slot(state, resume.entry.session);
	if (existing == -2)
		return result;
	if (existing >= 0)
	{
		telemetry_session_slot &slot = state->slots[static_cast<std::size_t>(existing)];
		if (slot.has_resume_snapshot != 0U &&
		    enter_equal(slot.entry_snapshot, resume.entry) &&
		    handoff_equal(slot.resume_handoff_snapshot, resume.handoff))
		{
			result.outcome = telemetry_session_state_outcome::idempotent;
			result_from_slot(result, slot);
		}
		return result;
	}
	if (resume.entry.connection.connection_seq <= state->last_connection_sequence ||
	    (absent && resume.entry.session.id.session_seq <= state->last_local_session_sequence))
		return result;
	if (absent)
	{
		std::size_t index = state->max_slots;
		if (!prepare_new_slot(state, resume.entry.at_monotonic_usec,
				      resume.entry.at_utc_usec, result, index))
		{
			result.outcome = telemetry_session_state_outcome::capacity_full;
			return result;
		}
		telemetry_session_slot &slot = state->slots[index];
		initialize_slot(slot, resume.entry, telemetry_cumulative_counters{}, 0U,
				resume.entry.quality_flags | TELEMETRY_QUALITY_UNCLOSED_TAIL);
		slot.has_resume_snapshot = 1U;
		slot.resume_handoff_snapshot = resume.handoff;
		occupy_slot(state, index);
		const telemetry_session_ref no_session = ZERO_SESSION_REF;
		const telemetry_connection_id no_connection = ZERO_CONNECTION_ID;
		(void)emit_unclosed_gap(state, result, resume.entry.at_utc_usec, no_session,
					no_connection);
		telemetry_session_state_result entered = emit_lifecycle(
			state, slot, telemetry_lifecycle_kind::session_entered, slot.connection,
			telemetry_session_end_reason::unknown, resume.entry.at_monotonic_usec,
			resume.entry.at_utc_usec, TELEMETRY_QUALITY_UNCLOSED_TAIL);
		result_merge(result, entered);
		result.outcome =
			entered.outcome == telemetry_session_state_outcome::accepted ?
				telemetry_session_state_outcome::accepted_unclosed_recovery :
				entered.outcome;
		result_from_slot(result, slot);
		result.quality_flags |= TELEMETRY_QUALITY_UNCLOSED_TAIL;
		return result;
	}
	std::size_t index = state->max_slots;
	if (!prepare_new_slot(state, resume.entry.at_monotonic_usec, resume.entry.at_utc_usec,
			      result, index))
	{
		result.outcome = telemetry_session_state_outcome::capacity_full;
		return result;
	}
	telemetry_session_slot &slot = state->slots[index];
	initialize_slot(slot, resume.entry, resume.handoff.cumulative,
			resume.handoff.last_checkpoint_revision,
			resume.handoff.quality_flags | resume.entry.quality_flags);
	slot.has_resume_snapshot = 1U;
	slot.resume_handoff_snapshot = resume.handoff;
	occupy_slot(state, index);
	telemetry_session_state_result attached =
		emit_lifecycle(state, slot, telemetry_lifecycle_kind::connection_attached,
			       slot.connection, telemetry_session_end_reason::unknown,
			       resume.entry.at_monotonic_usec, resume.entry.at_utc_usec,
			       resume.handoff.quality_flags | resume.entry.quality_flags);
	result_merge(result, attached);
	result.outcome = attached.outcome;
	result_from_slot(result, slot);
	return result;
}

telemetry_session_state_result
telemetry_session_state_retire_expired(telemetry_session_state *state) noexcept
{
	telemetry_session_state_result result =
		result_with(telemetry_session_state_outcome::retired);
	if (!state_ready(state))
	{
		result.outcome = telemetry_session_state_outcome::invalid;
		return result;
	}
	telemetry_monotonic_usec now_monotonic_usec = 0U;
	telemetry_utc_usec now_utc_usec = TELEMETRY_UTC_UNKNOWN;
	if (state->clock.now == nullptr ||
	    !state->clock.now(state->clock.context, &now_monotonic_usec, &now_utc_usec))
	{
		result.outcome = telemetry_session_state_outcome::clock_unavailable;
		return result;
	}
	for (std::size_t count = 0; count < state->max_slots; ++count)
	{
		const std::size_t index = find_expired_detached_slot(state, now_monotonic_usec);
		if (index >= state->max_slots)
			break;
		telemetry_session_state_result retired = retire_one_at(state, index, now_utc_usec);
		result_merge(result, retired);
	}
	if (result.slots_retired == 0U)
		result.outcome = telemetry_session_state_outcome::idempotent;
	else
		result.outcome = telemetry_session_state_outcome::retired;
	return result;
}

bool telemetry_session_state_copy_view(const telemetry_session_state *state,
				       telemetry_session_ref session,
				       telemetry_session_state_view *view) noexcept
{
	if (view != nullptr)
		*view = {};
	if (!state_ready(state) || view == nullptr)
		return false;
	const int found = find_slot(state, session);
	if (found < 0)
		return false;
	const telemetry_session_slot &slot = state->slots[static_cast<std::size_t>(found)];
	view->session = slot.session;
	view->connection = slot.connected != 0U ? slot.connection : ZERO_CONNECTION_ID;
	view->cumulative = slot.cumulative;
	view->last_allocated_revision = slot.last_allocated_revision;
	view->quality_flags = slot.quality_flags;
	view->connected = slot.connected;
	view->closed = slot.lifecycle == telemetry_session_slot_lifecycle::closed ? 1U : 0U;
	return true;
}

telemetry_session_state_stats
telemetry_session_state_stats_copy(const telemetry_session_state *state) noexcept
{
	telemetry_session_state_stats stats{};
	if (!state_ready(state))
		return stats;
	stats.capacity = state->max_slots;
	stats.used_slots = state->used_slots;
	stats.resident_slots = state->resident_slots;
	stats.closed_slots = state->closed_slots;
	stats.accepted_control_total = state->accepted_control_total;
	stats.dropped_control_total = state->dropped_control_total;
	stats.retired_session_total = state->retired_session_total;
	stats.unclosed_tail_total = state->unclosed_tail_total;
	stats.revision_exhaustion_total = state->revision_exhaustion_total;
	return stats;
}
