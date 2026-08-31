#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "prototypes.h"
#include "structs.h"
#include "utils.h"

#define NEVENT_PERIODIC_MAX_JOBS 32
#define NEVENT_PERIODIC_WATCHDOG_RETRY 4ULL

struct nevent_periodic_job_state
{
	char key[NEVENT_PERIODIC_KEY_LENGTH];
	char last_failure[NEVENT_PERIODIC_FAILURE_LENGTH];
	event_func_type callback;
	nevent_periodic_policy policy;
	bool configured;
	bool enabled;
	bool arming;
	bool disarming;
	bool running;
	bool run_failed;
	bool run_continuation;
	bool next_delay_overridden;
	bool has_started;
	bool has_succeeded;
	unsigned long long initial_delay;
	unsigned long long interval;
	unsigned long long next_delay;
	unsigned long long next_due_tick;
	unsigned long long next_watchdog_tick;
	unsigned long long last_started_tick;
	unsigned long long last_success_tick;
	unsigned long long total_runs;
	unsigned long long completed_runs;
	unsigned long long continuation_slices;
	unsigned long long callback_failures;
	unsigned long long consecutive_failures;
	unsigned long long schedule_failures;
	unsigned long long missed_runs;
	unsigned long long watchdog_rearms;
	unsigned long long duplicates_suppressed;
	nevent_handle handle;
	P_nevent running_event;
};

static nevent_periodic_job_state periodic_jobs[NEVENT_PERIODIC_MAX_JOBS];
static size_t periodic_job_count = 0;
static nevent_periodic_job_state *periodic_current_job = NULL;

extern unsigned long long ne_event_tick;

static unsigned long long periodic_add_ticks(unsigned long long tick, unsigned long long delay)
{
	if (delay > ULLONG_MAX - tick)
		return ULLONG_MAX;
	return tick + delay;
}

static void periodic_copy_failure(nevent_periodic_job_state *job, const char *reason)
{
	if (!job)
		return;
	snprintf(job->last_failure, sizeof(job->last_failure), "%s",
		 reason && *reason ? reason : "unspecified failure");
}

static nevent_periodic_job_state *periodic_find_job(const char *key)
{
	if (!key)
		return NULL;
	for (size_t index = 0; index < periodic_job_count; ++index)
		if (!strcmp(periodic_jobs[index].key, key))
			return &periodic_jobs[index];
	return NULL;
}

static unsigned int periodic_job_id(const nevent_periodic_job_state *job)
{
	return static_cast<unsigned int>(job - periodic_jobs) + 1;
}

static bool periodic_handle_matches(const nevent_periodic_job_state *job)
{
	return job && nevent_handle_is_active(job->handle) &&
	       job->handle.event->periodic_job_id == periodic_job_id(job) &&
	       job->handle.event->func == job->callback;
}

static void periodic_note_schedule_failure(nevent_periodic_job_state *job,
					   nevent_schedule_status status)
{
	job->schedule_failures++;
	job->next_watchdog_tick = periodic_add_ticks(ne_event_tick, NEVENT_PERIODIC_WATCHDOG_RETRY);
	snprintf(job->last_failure, sizeof(job->last_failure), "schedule status %u",
		 static_cast<unsigned int>(status));
	logit(LOG_SYS, "NEVENT PERIODIC: key=%s could not arm (status=%u)", job->key,
	      static_cast<unsigned int>(status));
}

static bool periodic_schedule_at(nevent_periodic_job_state *job, unsigned long long due_tick)
{
	if (!job || !job->enabled)
		return false;
	if (due_tick <= ne_event_tick)
		due_tick = periodic_add_ticks(ne_event_tick, 1);
	if (due_tick == ULLONG_MAX || due_tick - ne_event_tick > INT_MAX)
	{
		periodic_note_schedule_failure(job, nevent_schedule_status::negative_delay);
		return false;
	}

	job->arming = true;
	const nevent_schedule_result scheduled =
		add_event(job->callback, static_cast<int>(due_tick - ne_event_tick), NULL, NULL,
			  NULL, 0, NULL, 0);
	job->arming = false;
	if (!scheduled)
	{
		periodic_note_schedule_failure(job, scheduled.status);
		return false;
	}

	job->handle = scheduled.handle;
	job->handle.event->periodic_job_id = periodic_job_id(job);
	job->next_due_tick = job->handle.event->due_tick;
	job->next_watchdog_tick = 0;
	return true;
}

static unsigned long long periodic_fixed_rate_due(nevent_periodic_job_state *job)
{
	unsigned long long due_tick = periodic_add_ticks(job->next_due_tick, job->interval);

	if (due_tick > ne_event_tick)
		return due_tick;
	if (due_tick == ULLONG_MAX)
		return due_tick;

	const unsigned long long missed = (ne_event_tick - due_tick) / job->interval + 1;
	job->missed_runs += missed;
	if (missed > (ULLONG_MAX - due_tick) / job->interval)
		return ULLONG_MAX;
	return due_tick + missed * job->interval;
}

nevent_periodic_result nevent_periodic_register(const char *key, event_func_type callback,
						unsigned long long initial_delay,
						unsigned long long interval,
						nevent_periodic_policy policy, bool enabled)
{
	if (!nevent_require_game_thread("nevent_periodic_register"))
		return nevent_periodic_result::wrong_thread;
	if (!key || !*key || strlen(key) >= NEVENT_PERIODIC_KEY_LENGTH || !callback ||
	    initial_delay == 0 || initial_delay > INT_MAX || interval == 0 || interval > INT_MAX)
		return nevent_periodic_result::invalid_definition;

	if (nevent_periodic_job_state *existing = periodic_find_job(key))
	{
		if (existing->callback != callback || existing->initial_delay != initial_delay ||
		    existing->interval != interval || existing->policy != policy ||
		    existing->enabled != enabled)
		{
			periodic_copy_failure(existing, "conflicting duplicate registration");
			return nevent_periodic_result::invalid_definition;
		}
		existing->duplicates_suppressed++;
		return nevent_periodic_result::duplicate_suppressed;
	}

	for (size_t index = 0; index < periodic_job_count; ++index)
		if (periodic_jobs[index].callback == callback)
		{
			periodic_copy_failure(&periodic_jobs[index],
					      "callback registered under another key");
			return nevent_periodic_result::invalid_definition;
		}
	if (periodic_job_count >= NEVENT_PERIODIC_MAX_JOBS)
		return nevent_periodic_result::capacity_exhausted;

	nevent_periodic_job_state *job = &periodic_jobs[periodic_job_count++];
	memset(job, 0, sizeof(*job));
	snprintf(job->key, sizeof(job->key), "%s", key);
	job->callback = callback;
	job->policy = policy;
	job->configured = true;
	job->enabled = enabled;
	job->initial_delay = initial_delay;
	job->interval = interval;
	job->next_due_tick = periodic_add_ticks(ne_event_tick, initial_delay);

	if (enabled && !periodic_schedule_at(job, job->next_due_tick))
		return nevent_periodic_result::schedule_failed;
	return nevent_periodic_result::registered;
}

nevent_periodic_result nevent_periodic_set_enabled(const char *key, bool enabled,
						   unsigned long long initial_delay)
{
	if (!nevent_require_game_thread("nevent_periodic_set_enabled"))
		return nevent_periodic_result::wrong_thread;
	nevent_periodic_job_state *job = periodic_find_job(key);
	if (!job)
		return nevent_periodic_result::unknown_key;
	if (job->enabled == enabled)
	{
		job->duplicates_suppressed++;
		return nevent_periodic_result::duplicate_suppressed;
	}

	if (!enabled)
	{
		job->enabled = false;
		if (nevent_handle_is_active(job->handle))
		{
			job->disarming = true;
			nevent_cancel(job->handle);
		}
		else
			job->handle = {};
		return nevent_periodic_result::disabled;
	}

	if (initial_delay == 0 || initial_delay > INT_MAX)
		return nevent_periodic_result::invalid_definition;
	job->enabled = true;
	job->initial_delay = initial_delay;
	job->next_due_tick = periodic_add_ticks(ne_event_tick, initial_delay);
	if (!periodic_schedule_at(job, job->next_due_tick))
		return nevent_periodic_result::schedule_failed;
	return nevent_periodic_result::enabled;
}

void nevent_periodic_reset()
{
	if (!nevent_require_game_thread("nevent_periodic_reset"))
		return;
	memset(periodic_jobs, 0, sizeof(periodic_jobs));
	periodic_job_count = 0;
	periodic_current_job = NULL;
}

bool nevent_periodic_begin(P_nevent event)
{
	if (!nevent_require_game_thread("nevent_periodic_begin"))
		return false;
	if (!event || event->periodic_job_id == 0)
		return false;
	const size_t index = event->periodic_job_id - 1;
	if (index >= periodic_job_count)
		panic_corruption("nevent_periodic", "event sequence %llu has invalid job id %u",
				 event->sequence, event->periodic_job_id);
	nevent_periodic_job_state *job = &periodic_jobs[index];
	if (!job->configured || job->callback != event->func || job->running ||
	    periodic_current_job || job->handle.event != event ||
	    job->handle.sequence != event->sequence)
		panic_corruption("nevent_periodic",
				 "event sequence %llu does not own periodic key %s",
				 event->sequence, job->key);

	job->handle = {};
	job->running = true;
	job->running_event = event;
	job->run_failed = false;
	job->run_continuation = false;
	job->next_delay_overridden = false;
	job->has_started = true;
	job->last_started_tick = ne_event_tick;
	job->total_runs++;
	periodic_current_job = job;
	return true;
}

void nevent_periodic_complete(P_nevent event)
{
	if (!nevent_require_game_thread("nevent_periodic_complete"))
		return;
	if (!event || !periodic_current_job || !periodic_current_job->running ||
	    periodic_current_job->running_event != event)
		panic_corruption("nevent_periodic", "completion does not match the running job");

	nevent_periodic_job_state *job = periodic_current_job;
	if (job->run_continuation)
		job->continuation_slices++;
	if (job->run_failed)
	{
		job->callback_failures++;
		job->consecutive_failures++;
	}
	else if (!job->run_continuation)
	{
		job->has_succeeded = true;
		job->last_success_tick = ne_event_tick;
		job->consecutive_failures = 0;
		job->completed_runs++;
	}

	if (!job->enabled)
	{
		job->running = false;
		job->running_event = NULL;
		periodic_current_job = NULL;
		return;
	}

	unsigned long long due_tick;
	if (job->next_delay_overridden)
		due_tick = periodic_add_ticks(ne_event_tick, job->next_delay);
	else if (job->policy == nevent_periodic_policy::fixed_rate)
		due_tick = periodic_fixed_rate_due(job);
	else
		due_tick = periodic_add_ticks(ne_event_tick, job->interval);
	job->next_due_tick = due_tick;
	periodic_schedule_at(job, due_tick);
	job->running = false;
	job->running_event = NULL;
	periodic_current_job = NULL;
}

void nevent_periodic_mark_failure(const char *reason)
{
	if (!nevent_require_game_thread("nevent_periodic_mark_failure"))
		return;
	if (!periodic_current_job)
		panic_corruption("nevent_periodic", "failure reported outside a periodic callback");
	periodic_current_job->run_failed = true;
	periodic_copy_failure(periodic_current_job, reason);
}

void nevent_periodic_retry_after(unsigned long long delay, const char *reason)
{
	if (!nevent_require_game_thread("nevent_periodic_retry_after"))
		return;
	if (!periodic_current_job || delay == 0 || delay > INT_MAX)
		panic_corruption("nevent_periodic", "invalid periodic retry delay %llu", delay);
	nevent_periodic_mark_failure(reason);
	periodic_current_job->next_delay = delay;
	periodic_current_job->next_delay_overridden = true;
}

void nevent_periodic_next_after(unsigned long long delay)
{
	if (!nevent_require_game_thread("nevent_periodic_next_after"))
		return;
	if (!periodic_current_job || delay == 0 || delay > INT_MAX)
		panic_corruption("nevent_periodic", "invalid periodic successor delay %llu", delay);
	periodic_current_job->next_delay = delay;
	periodic_current_job->next_delay_overridden = true;
}

void nevent_periodic_continue_after(unsigned long long delay)
{
	if (!nevent_require_game_thread("nevent_periodic_continue_after"))
		return;
	if (!periodic_current_job || delay == 0 || delay > INT_MAX)
		panic_corruption("nevent_periodic", "invalid periodic continuation delay %llu",
				 delay);
	periodic_current_job->run_continuation = true;
	periodic_current_job->next_delay = delay;
	periodic_current_job->next_delay_overridden = true;
}

void nevent_periodic_watchdog()
{
	if (!nevent_require_game_thread("nevent_periodic_watchdog"))
		return;
	for (size_t index = 0; index < periodic_job_count; ++index)
	{
		nevent_periodic_job_state *job = &periodic_jobs[index];
		if (job->disarming)
		{
			if (nevent_handle_is_active(job->handle))
				continue;
			job->handle = {};
			job->disarming = false;
		}
		if (!job->enabled || job->running || job->arming)
			continue;
		if (periodic_handle_matches(job))
		{
			job->next_due_tick = job->handle.event->due_tick;
			continue;
		}
		if (nevent_handle_is_active(job->handle))
			panic_corruption("nevent_periodic", "key %s has a mismatched live event",
					 job->key);
		job->handle = {};
		if (job->next_watchdog_tick > ne_event_tick)
			continue;

		unsigned long long due_tick = job->next_due_tick;
		if (due_tick <= ne_event_tick)
		{
			if (job->policy == nevent_periodic_policy::fixed_rate && due_tick &&
			    job->interval)
			{
				const unsigned long long missed =
					(ne_event_tick - due_tick) / job->interval + 1;
				job->missed_runs += missed;
				if (missed > (ULLONG_MAX - due_tick) / job->interval)
					due_tick = ULLONG_MAX;
				else
					due_tick += missed * job->interval;
			}
			else
			{
				job->missed_runs++;
				due_tick = periodic_add_ticks(ne_event_tick, 1);
			}
		}
		job->watchdog_rearms++;
		job->next_due_tick = due_tick;
		periodic_schedule_at(job, due_tick);
	}
}

size_t nevent_periodic_copy_health(nevent_periodic_health *health, size_t capacity)
{
	if (!nevent_require_game_thread("nevent_periodic_copy_health"))
		return 0;
	if (!health || capacity == 0)
		return periodic_job_count;
	const size_t copied = capacity < periodic_job_count ? capacity : periodic_job_count;
	for (size_t index = 0; index < copied; ++index)
	{
		const nevent_periodic_job_state *job = &periodic_jobs[index];
		nevent_periodic_health *item = &health[index];
		memset(item, 0, sizeof(*item));
		memcpy(item->key, job->key, sizeof(item->key));
		memcpy(item->last_failure, job->last_failure, sizeof(item->last_failure));
		item->callback = job->callback;
		item->policy = job->policy;
		item->enabled = job->enabled;
		item->armed = periodic_handle_matches(job);
		item->running = job->running;
		item->has_started = job->has_started;
		item->has_succeeded = job->has_succeeded;
		item->initial_delay = job->initial_delay;
		item->interval = job->interval;
		item->next_due_tick = item->armed ? job->handle.event->due_tick :
						    job->next_due_tick;
		item->last_started_tick = job->last_started_tick;
		item->last_success_tick = job->last_success_tick;
		item->total_runs = job->total_runs;
		item->completed_runs = job->completed_runs;
		item->continuation_slices = job->continuation_slices;
		item->callback_failures = job->callback_failures;
		item->consecutive_failures = job->consecutive_failures;
		item->schedule_failures = job->schedule_failures;
		item->missed_runs = job->missed_runs;
		item->watchdog_rearms = job->watchdog_rearms;
		item->duplicates_suppressed = job->duplicates_suppressed;
	}
	return periodic_job_count;
}

nevent_periodic_summary nevent_periodic_summary_copy()
{
	nevent_periodic_summary summary = {};
	if (!nevent_require_game_thread("nevent_periodic_summary_copy"))
		return summary;
	summary.registered = static_cast<unsigned long>(periodic_job_count);
	for (size_t index = 0; index < periodic_job_count; ++index)
	{
		const nevent_periodic_job_state *job = &periodic_jobs[index];
		const bool armed = periodic_handle_matches(job);
		if (job->enabled)
			summary.enabled++;
		if (armed)
			summary.armed++;
		if (job->running)
			summary.running++;
		if (job->enabled && !armed && !job->running)
			summary.unhealthy++;
		summary.callback_failures += job->callback_failures;
		summary.schedule_failures += job->schedule_failures;
		summary.missed_runs += job->missed_runs;
		summary.watchdog_rearms += job->watchdog_rearms;
		summary.duplicates_suppressed += job->duplicates_suppressed;
	}
	return summary;
}

bool nevent_periodic_event_is_valid(P_nevent event)
{
	if (!nevent_require_game_thread("nevent_periodic_event_is_valid"))
		return false;
	if (!event || event->periodic_job_id == 0)
		return true;
	const size_t index = event->periodic_job_id - 1;
	if (index >= periodic_job_count)
		return false;
	const nevent_periodic_job_state *job = &periodic_jobs[index];
	if (!job->configured || event->func != job->callback)
		return false;
	if (job->running)
		return job->running_event == event;
	return job->handle.event == event && job->handle.sequence == event->sequence;
}

long nevent_periodic_integrity_errors(bool emit)
{
	if (!nevent_require_game_thread("nevent_periodic_integrity_errors"))
		return 1;
	long errors = 0;
	for (size_t index = 0; index < periodic_job_count; ++index)
	{
		const nevent_periodic_job_state *job = &periodic_jobs[index];
		if (job->arming || job->disarming)
			continue;
		const bool armed = periodic_handle_matches(job);
		if (job->enabled && !armed && !job->running)
		{
			errors++;
			if (emit)
				logit(LOG_SYS, "NEVENT PERIODIC INTEGRITY: key=%s has no successor",
				      job->key);
		}
		if (!job->enabled && (armed || job->running))
		{
			errors++;
			if (emit)
				logit(LOG_SYS,
				      "NEVENT PERIODIC INTEGRITY: disabled key=%s is still active",
				      job->key);
		}
		if (armed && !nevent_periodic_event_is_valid(job->handle.event))
		{
			errors++;
			if (emit)
				logit(LOG_SYS,
				      "NEVENT PERIODIC INTEGRITY: key=%s has invalid event metadata",
				      job->key);
		}
	}
	return errors;
}
