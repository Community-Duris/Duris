#include "telemetry/telemetry_health.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace
{

telemetry_duration_usec age_at(telemetry_monotonic_usec now, telemetry_monotonic_usec then) noexcept
{
	return then != 0U && now >= then ? now - then : 0U;
}

std::uint64_t saturating_add(std::uint64_t left, std::uint64_t right) noexcept
{
	return right > std::numeric_limits<std::uint64_t>::max() - left ?
		       std::numeric_limits<std::uint64_t>::max() :
		       left + right;
}

std::uint64_t admitted_count(const telemetry_health_snapshot &health) noexcept
{
	return saturating_add(health.admitted_detail, health.admitted_control);
}

std::uint64_t completed_count(const telemetry_health_snapshot &health) noexcept
{
	std::uint64_t completed = saturating_add(health.applied_records, health.duplicate_records);
	completed = saturating_add(completed, health.stale_checkpoint_records);
	completed = saturating_add(completed, health.invalid_records);
	return saturating_add(completed, health.conflict_records);
}

bool failure_is_permanent(telemetry_failure_class failure) noexcept
{
	return failure == telemetry_failure_class::permanent_schema ||
	       failure == telemetry_failure_class::permanent_permission ||
	       failure == telemetry_failure_class::permanent_repository;
}

telemetry_health_failure_signature
failure_signature(const telemetry_health_snapshot &health) noexcept
{
	telemetry_health_failure_signature signature{};
	signature.failure_class = health.last_failure_class;
	signature.error_code = health.last_error_code;
	signature.producer = health.last_failure_producer;
	signature.first_record_seq = health.last_failure_first_record_seq;
	signature.last_record_seq = health.last_failure_last_record_seq;
	signature.record_kind_mask = health.last_failure_record_kind_mask;
	return signature;
}

bool same_producer(telemetry_producer_id left, telemetry_producer_id right) noexcept
{
	return left.boot_id == right.boot_id && left.process_id == right.process_id;
}

bool same_failure(const telemetry_health_failure_signature &left,
		  const telemetry_health_failure_signature &right) noexcept
{
	return left.failure_class == right.failure_class && left.error_code == right.error_code &&
	       same_producer(left.producer, right.producer) &&
	       left.first_record_seq == right.first_record_seq &&
	       left.last_record_seq == right.last_record_seq &&
	       left.record_kind_mask == right.record_kind_mask;
}

bool queue_under_pressure(const telemetry_health_monitor_config &config,
			  const telemetry_health_snapshot &health) noexcept
{
	const std::uint64_t capacity = config.queue_capacity != 0U ? config.queue_capacity :
								     health.queue_capacity;
	if (capacity == 0U || config.queue_pressure_percent == 0U)
		return false;
	const std::uint64_t threshold = (capacity * config.queue_pressure_percent + 99U) / 100U;
	return health.queue_depth >= threshold;
}

void choose_affected_range(const telemetry_health_snapshot &health, telemetry_producer_id *producer,
			   telemetry_record_sequence *first,
			   telemetry_record_sequence *last) noexcept
{
	*producer = health.producer;
	*first = 0U;
	*last = 0U;
	if (health.inflight_active != 0U && health.inflight_first_record_seq != 0U)
	{
		*first = health.inflight_first_record_seq;
		*last = health.inflight_last_record_seq;
		return;
	}
	if (health.last_failure_first_record_seq != 0U)
	{
		*producer = telemetry_producer_id_is_valid(health.last_failure_producer) ?
				    health.last_failure_producer :
				    health.producer;
		*first = health.last_failure_first_record_seq;
		*last = health.last_failure_last_record_seq;
		return;
	}
	if (health.last_admitted_record_seq > health.last_committed_record_seq)
	{
		*first = health.last_committed_record_seq ==
					 std::numeric_limits<telemetry_record_sequence>::max() ?
				 0U :
				 health.last_committed_record_seq + 1U;
		*last = health.last_admitted_record_seq;
	}
}

void begin_or_extend_affected_range(telemetry_health_monitor_state &state,
				    const telemetry_health_snapshot &health,
				    bool beginning) noexcept
{
	telemetry_producer_id producer{};
	telemetry_record_sequence first = 0U;
	telemetry_record_sequence last = 0U;
	choose_affected_range(health, &producer, &first, &last);
	if (beginning || !same_producer(state.affected_producer, producer))
	{
		state.affected_producer = producer;
		state.affected_first_record_seq = first;
		state.affected_last_record_seq = last;
		return;
	}
	if (state.affected_first_record_seq == 0U ||
	    (first != 0U && first < state.affected_first_record_seq))
		state.affected_first_record_seq = first;
	if (last > state.affected_last_record_seq)
		state.affected_last_record_seq = last;
}

void fill_ages(telemetry_health_event &event, telemetry_monotonic_usec now) noexcept
{
	if (event.health.last_success_monotonic_usec != 0U &&
	    now >= event.health.last_success_monotonic_usec)
	{
		event.last_commit_age_available = 1U;
		event.last_commit_age_usec = now - event.health.last_success_monotonic_usec;
	}
	if (event.health.last_failure_monotonic_usec != 0U &&
	    now >= event.health.last_failure_monotonic_usec)
	{
		event.last_failure_age_available = 1U;
		event.last_failure_age_usec = now - event.health.last_failure_monotonic_usec;
	}
}

telemetry_health_event make_event(const telemetry_health_monitor_state &state,
				  telemetry_health_event_kind kind,
				  telemetry_health_alert_severity severity,
				  telemetry_health_state previous, telemetry_health_snapshot health,
				  telemetry_monotonic_usec now) noexcept
{
	telemetry_health_event event{};
	event.kind = kind;
	event.severity = severity;
	event.previous_state = previous;
	event.current_state = health.state;
	event.reason_mask = state.active_reason_mask;
	event.alert_duration_usec = age_at(now, state.alert_started_usec);
	event.affected_producer = state.affected_producer;
	event.affected_first_record_seq = state.affected_first_record_seq;
	event.affected_last_record_seq = state.affected_last_record_seq;
	event.health = health;
	fill_ages(event, now);
	return event;
}

std::size_t append_text(char *output, std::size_t capacity, std::size_t used,
			const char *text) noexcept
{
	if (output == nullptr || capacity == 0U || text == nullptr)
		return used;
	while (*text != '\0' && used + 1U < capacity)
		output[used++] = *text++;
	output[used] = '\0';
	return used;
}

} // namespace

telemetry_health_monitor_config
telemetry_health_monitor_default_config(telemetry_duration_usec interval_usec,
					std::uint32_t queue_capacity) noexcept
{
	telemetry_health_monitor_config config{};
	config.interval_usec = interval_usec == 0U ? TELEMETRY_INTERVAL_USEC_PROPOSAL :
						     interval_usec;
	config.critical_after_usec = TELEMETRY_HEALTH_CRITICAL_AFTER_USEC;
	config.repeat_after_usec = TELEMETRY_HEALTH_REPEAT_AFTER_USEC;
	config.queue_capacity = queue_capacity;
	config.queue_pressure_percent = TELEMETRY_HEALTH_QUEUE_PRESSURE_PERCENT;
	return config;
}

void telemetry_health_monitor_reset(telemetry_health_monitor_state *state) noexcept
{
	if (state != nullptr)
		*state = {};
}

telemetry_health_event telemetry_health_monitor_evaluate(
	telemetry_health_monitor_state *state, const telemetry_health_monitor_config *config,
	telemetry_health_snapshot health, telemetry_monotonic_usec now) noexcept
{
	telemetry_health_event none{};
	if (state == nullptr || config == nullptr)
		return none;

	const bool first_observation = state->initialized == 0U;
	const telemetry_health_state previous_state = first_observation ? health.state :
									  state->last_state;
	const bool state_changed = !first_observation && previous_state != health.state;
	const telemetry_health_failure_signature current_failure = failure_signature(health);
	const bool failure_changed = !same_failure(state->last_failure, current_failure);
	state->last_failure = current_failure;

	if (health.dropped_control > state->last_control_drop_count)
	{
		state->control_drop_pending = 1U;
		state->control_drop_success_marker = health.last_success_monotonic_usec;
	}
	state->last_control_drop_count = health.dropped_control;
	if (state->control_drop_pending != 0U && health.state == telemetry_health_state::healthy &&
	    health.last_success_monotonic_usec > state->control_drop_success_marker)
		state->control_drop_pending = 0U;

	const std::uint64_t admitted = admitted_count(health);
	const bool outstanding = admitted > completed_count(health);
	if (!outstanding)
		state->pending_since_usec = 0U;
	else if (state->pending_since_usec == 0U)
		state->pending_since_usec = now;
	telemetry_monotonic_usec progress_anchor = state->pending_since_usec;
	if (health.last_success_monotonic_usec > progress_anchor)
		progress_anchor = health.last_success_monotonic_usec;
	const telemetry_duration_usec stalled_for = outstanding ? age_at(now, progress_anchor) : 0U;

	std::uint32_t reasons = 0U;
	telemetry_health_alert_severity severity = telemetry_health_alert_severity::none;
	if (health.state == telemetry_health_state::degraded)
	{
		reasons |= TELEMETRY_HEALTH_REASON_WRITER_DEGRADED;
		severity = telemetry_health_alert_severity::warning;
	}
	if (health.state == telemetry_health_state::circuit_open)
	{
		reasons |= TELEMETRY_HEALTH_REASON_CIRCUIT_OPEN;
		severity = telemetry_health_alert_severity::critical;
	}
	if (failure_is_permanent(health.last_failure_class) &&
	    health.last_failure_monotonic_usec != 0U &&
	    health.state != telemetry_health_state::healthy &&
	    (health.last_success_monotonic_usec == 0U ||
	     health.last_failure_monotonic_usec >= health.last_success_monotonic_usec))
	{
		reasons |= TELEMETRY_HEALTH_REASON_PERMANENT_FAILURE;
		severity = telemetry_health_alert_severity::critical;
	}
	if (queue_under_pressure(*config, health))
	{
		reasons |= TELEMETRY_HEALTH_REASON_QUEUE_PRESSURE;
		severity = telemetry_health_alert_severity::critical;
	}
	if (state->control_drop_pending != 0U)
	{
		reasons |= TELEMETRY_HEALTH_REASON_CONTROL_DROP;
		severity = telemetry_health_alert_severity::critical;
	}
	const telemetry_duration_usec warning_after =
		config->interval_usec > std::numeric_limits<telemetry_duration_usec>::max() / 2U ?
			std::numeric_limits<telemetry_duration_usec>::max() :
			config->interval_usec * 2U;
	if (outstanding &&
	    (stalled_for >= warning_after || stalled_for >= config->critical_after_usec))
	{
		reasons |= TELEMETRY_HEALTH_REASON_STALLED;
		severity = stalled_for >= config->critical_after_usec ?
				   telemetry_health_alert_severity::critical :
				   std::max(severity, telemetry_health_alert_severity::warning);
	}
	const std::uint32_t failure_reasons = TELEMETRY_HEALTH_REASON_WRITER_DEGRADED |
					      TELEMETRY_HEALTH_REASON_CIRCUIT_OPEN |
					      TELEMETRY_HEALTH_REASON_PERMANENT_FAILURE |
					      TELEMETRY_HEALTH_REASON_RECOVERY_PENDING;
	if (severity == telemetry_health_alert_severity::none &&
	    (state->active_reason_mask & failure_reasons) != 0U &&
	    (health.last_success_monotonic_usec == 0U ||
	     health.last_success_monotonic_usec <= health.last_failure_monotonic_usec))
	{
		reasons = TELEMETRY_HEALTH_REASON_RECOVERY_PENDING;
		severity = state->active_severity;
	}

	state->initialized = 1U;
	state->last_state = health.state;
	if (severity != telemetry_health_alert_severity::none)
	{
		const bool beginning = state->active_severity ==
				       telemetry_health_alert_severity::none;
		const bool alert_changed = beginning || state->active_severity != severity ||
					   state->active_reason_mask != reasons || state_changed ||
					   failure_changed;
		if (beginning)
		{
			state->alert_started_usec = now;
			state->affected_producer = {};
			state->affected_first_record_seq = 0U;
			state->affected_last_record_seq = 0U;
		}
		state->active_severity = severity;
		state->active_reason_mask = reasons;
		begin_or_extend_affected_range(*state, health, beginning);
		if (alert_changed)
		{
			state->last_emitted_usec = now;
			return make_event(*state, telemetry_health_event_kind::alert, severity,
					  previous_state, health, now);
		}
		if (config->repeat_after_usec != 0U &&
		    age_at(now, state->last_emitted_usec) >= config->repeat_after_usec)
		{
			state->last_emitted_usec = now;
			return make_event(*state, telemetry_health_event_kind::reminder, severity,
					  previous_state, health, now);
		}
		return none;
	}

	if (state->active_severity != telemetry_health_alert_severity::none)
	{
		telemetry_health_event event = make_event(
			*state,
			health.state == telemetry_health_state::healthy ?
				telemetry_health_event_kind::recovery :
				telemetry_health_event_kind::state,
			telemetry_health_alert_severity::none, previous_state, health, now);
		state->active_severity = telemetry_health_alert_severity::none;
		state->active_reason_mask = 0U;
		state->alert_started_usec = 0U;
		state->last_emitted_usec = now;
		return event;
	}
	if (first_observation || state_changed)
	{
		state->last_emitted_usec = now;
		return make_event(*state, telemetry_health_event_kind::state,
				  telemetry_health_alert_severity::none, previous_state, health,
				  now);
	}
	return none;
}

telemetry_health_status telemetry_health_monitor_status_copy(
	const telemetry_health_monitor_state *state, const telemetry_health_monitor_config *config,
	telemetry_health_snapshot health, telemetry_monotonic_usec now) noexcept
{
	telemetry_health_status status{};
	status.health = health;
	if (config != nullptr)
		status.configured_interval_usec = config->interval_usec;
	if (state != nullptr)
	{
		status.active_severity = state->active_severity;
		status.active_reason_mask = state->active_reason_mask;
		status.active_alert_duration_usec = age_at(now, state->alert_started_usec);
	}
	if (health.last_success_monotonic_usec != 0U && now >= health.last_success_monotonic_usec)
	{
		status.last_commit_age_available = 1U;
		status.last_commit_age_usec = now - health.last_success_monotonic_usec;
	}
	if (health.last_failure_monotonic_usec != 0U && now >= health.last_failure_monotonic_usec)
	{
		status.last_failure_age_available = 1U;
		status.last_failure_age_usec = now - health.last_failure_monotonic_usec;
	}
	return status;
}

const char *telemetry_health_state_name(telemetry_health_state state) noexcept
{
	switch (state)
	{
	case telemetry_health_state::disabled:
		return "disabled";
	case telemetry_health_state::starting:
		return "starting";
	case telemetry_health_state::healthy:
		return "healthy";
	case telemetry_health_state::degraded:
		return "degraded";
	case telemetry_health_state::stopping:
		return "stopping";
	case telemetry_health_state::stopped:
		return "stopped";
	case telemetry_health_state::circuit_open:
		return "circuit-open";
	}
	return "unknown";
}

const char *telemetry_health_backend_name(telemetry_storage_backend backend) noexcept
{
	return backend == telemetry_storage_backend::sql	       ? "sql" :
	       backend == telemetry_storage_backend::flatfile_disabled ? "flatfile-disabled" :
									 "unknown";
}

const char *telemetry_health_failure_class_name(telemetry_failure_class failure) noexcept
{
	switch (failure)
	{
	case telemetry_failure_class::none:
		return "none";
	case telemetry_failure_class::transient_connection:
		return "transient-connection";
	case telemetry_failure_class::transient_transaction:
		return "transient-transaction";
	case telemetry_failure_class::transient_internal:
		return "transient-internal";
	case telemetry_failure_class::commit_ambiguous:
		return "commit-ambiguous";
	case telemetry_failure_class::invalid_record:
		return "invalid-record";
	case telemetry_failure_class::permanent_schema:
		return "permanent-schema";
	case telemetry_failure_class::permanent_permission:
		return "permanent-permission";
	case telemetry_failure_class::permanent_repository:
		return "permanent-repository";
	}
	return "unknown";
}

const char *telemetry_health_advisory_lock_name(telemetry_advisory_lock_state state) noexcept
{
	return state == telemetry_advisory_lock_state::held	      ? "held" :
	       state == telemetry_advisory_lock_state::unavailable    ? "unavailable" :
	       state == telemetry_advisory_lock_state::not_applicable ? "not-applicable" :
									"unknown";
}

const char *telemetry_health_alert_severity_name(telemetry_health_alert_severity severity) noexcept
{
	return severity == telemetry_health_alert_severity::critical ? "critical" :
	       severity == telemetry_health_alert_severity::warning  ? "warning" :
								       "none";
}

const char *telemetry_health_event_kind_name(telemetry_health_event_kind kind) noexcept
{
	switch (kind)
	{
	case telemetry_health_event_kind::none:
		return "none";
	case telemetry_health_event_kind::state:
		return "state";
	case telemetry_health_event_kind::alert:
		return "alert";
	case telemetry_health_event_kind::reminder:
		return "reminder";
	case telemetry_health_event_kind::recovery:
		return "recovery";
	}
	return "unknown";
}

std::size_t telemetry_health_record_kind_mask_format(std::uint64_t mask, char *output,
						     std::size_t capacity) noexcept
{
	if (output == nullptr || capacity == 0U)
		return 0U;
	output[0] = '\0';
	struct entry
	{
		telemetry_record_kind kind;
		const char *name;
	};
	static constexpr entry entries[] = {
		{ telemetry_record_kind::interval, "interval" },
		{ telemetry_record_kind::session_lifecycle, "session-lifecycle" },
		{ telemetry_record_kind::session_checkpoint, "session-checkpoint" },
		{ telemetry_record_kind::coverage_gap, "coverage-gap" },
		{ telemetry_record_kind::configuration, "configuration" },
		{ telemetry_record_kind::progression, "progression" },
		{ telemetry_record_kind::encounter, "encounter" },
		{ telemetry_record_kind::combat_summary, "combat-summary" },
	};
	std::size_t used = 0U;
	bool found = false;
	for (const auto &item : entries)
	{
		const std::uint64_t bit = std::uint64_t{ 1U }
					  << static_cast<std::uint8_t>(item.kind);
		if ((mask & bit) == 0U)
			continue;
		if (found)
			used = append_text(output, capacity, used, ",");
		used = append_text(output, capacity, used, item.name);
		found = true;
	}
	if (!found)
		used = append_text(output, capacity, used, "none");
	return used;
}

std::size_t telemetry_health_reason_mask_format(std::uint32_t mask, char *output,
						std::size_t capacity) noexcept
{
	if (output == nullptr || capacity == 0U)
		return 0U;
	output[0] = '\0';
	struct entry
	{
		std::uint32_t flag;
		const char *name;
	};
	static constexpr entry entries[] = {
		{ TELEMETRY_HEALTH_REASON_WRITER_DEGRADED, "writer-degraded" },
		{ TELEMETRY_HEALTH_REASON_STALLED, "stalled" },
		{ TELEMETRY_HEALTH_REASON_CIRCUIT_OPEN, "circuit-open" },
		{ TELEMETRY_HEALTH_REASON_QUEUE_PRESSURE, "queue-pressure" },
		{ TELEMETRY_HEALTH_REASON_CONTROL_DROP, "control-drop" },
		{ TELEMETRY_HEALTH_REASON_PERMANENT_FAILURE, "permanent-failure" },
		{ TELEMETRY_HEALTH_REASON_RECOVERY_PENDING, "recovery-pending" },
	};
	std::size_t used = 0U;
	bool found = false;
	for (const auto &item : entries)
	{
		if ((mask & item.flag) == 0U)
			continue;
		if (found)
			used = append_text(output, capacity, used, ",");
		used = append_text(output, capacity, used, item.name);
		found = true;
	}
	if (!found)
		used = append_text(output, capacity, used, "none");
	return used;
}
