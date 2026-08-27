#ifndef MAINTENANCE_SCHEDULER_H
#define MAINTENANCE_SCHEDULER_H

#include <cstddef>
#include <cstdint>
#include <array>

constexpr size_t MAINTENANCE_JOB_COUNT = 11;
constexpr size_t MAINTENANCE_QUEUE_MAX = MAINTENANCE_JOB_COUNT;
constexpr size_t MAINTENANCE_COMPLETION_MAX = MAINTENANCE_JOB_COUNT;
constexpr uint32_t MAINTENANCE_ROW_BUDGET_MAX = 256;
constexpr uint64_t MAINTENANCE_TIME_BUDGET_USEC_MAX = 50000;
constexpr uint64_t MAINTENANCE_RETRY_MAX_TICKS = 240;
constexpr size_t MAINTENANCE_VALUE_MAX = 1024;
constexpr size_t MAINTENANCE_CONTENT_MAX = 4096;

enum class maintenance_job_id : uint8_t
{
	auction_due_scan,
	poll_expiration,
	epic_task_catalog,
	epic_zone_balance,
	level_cap,
	zone_trophy,
	epic_zone_modifiers,
	boon_scan,
	web_status,
	cargo_market,
	operational_statistics,
};

enum class maintenance_outcome : uint8_t
{
	complete,
	more,
	retryable_failure,
	permanent_failure,
	cancelled,
};

struct maintenance_job_definition
{
	maintenance_job_id id;
	uint64_t cadence_ticks;
	uint8_t priority;
	uint32_t row_budget;
	uint64_t time_budget_usec;
	bool enabled;
};

struct maintenance_request
{
	uint64_t work_id;
	maintenance_job_id job_id;
	uint64_t cursor;
	uint64_t scheduled_tick;
	uint64_t deadline_usec;
	uint32_t row_budget;
	uint64_t time_budget_usec;
	size_t value_count = 0;
	std::array<int64_t, MAINTENANCE_VALUE_MAX> values = {};
	size_t content_size = 0;
	std::array<char, MAINTENANCE_CONTENT_MAX> content = {};
};

struct maintenance_result
{
	uint64_t work_id;
	maintenance_job_id job_id;
	maintenance_outcome outcome;
	uint64_t next_cursor;
	uint32_t rows;
	uint64_t run_usec;
	uint32_t error_code;
	size_t value_count;
	std::array<int64_t, MAINTENANCE_VALUE_MAX> values;
};

struct maintenance_job_health
{
	uint64_t offset_ticks;
	uint64_t next_due_tick;
	uint64_t cursor;
	uint64_t work_id;
	uint64_t submitted;
	uint64_t completed;
	uint64_t retries;
	uint64_t overlap_suppressed;
	uint64_t failures;
	uint64_t rows;
	uint64_t last_run_usec;
	bool active;
};

struct maintenance_scheduler_health
{
	maintenance_job_health jobs[MAINTENANCE_JOB_COUNT];
	uint64_t queued;
	uint64_t inflight;
	uint64_t completions;
	uint64_t high_water;
	uint64_t oldest_queue_age_ticks;
	uint64_t inflight_age_ticks;
	bool running;
	bool stop_pending;
};

using maintenance_execute_fn = maintenance_result (*)(const maintenance_request &, void *);
using maintenance_prepare_fn = bool (*)(maintenance_request &, void *);

const maintenance_job_definition *maintenance_registry(size_t *count);
const char *maintenance_job_name(maintenance_job_id id);
uint64_t maintenance_job_offset(maintenance_job_id id, uint64_t cadence_ticks,
				uint64_t instance_seed);
bool maintenance_activity_due(uint64_t tick, uint64_t cadence_ticks, uint64_t activity_id);
bool maintenance_scheduler_init(uint64_t instance_seed, maintenance_execute_fn execute,
				void *context = nullptr, maintenance_prepare_fn prepare = nullptr,
				void *prepare_context = nullptr);
bool maintenance_scheduler_set_state_path(const char *path);
size_t maintenance_scheduler_pulse(uint64_t tick, maintenance_result *results, size_t capacity);
void maintenance_scheduler_quiesce(void);
bool maintenance_scheduler_drain(uint64_t timeout_msec);
void maintenance_scheduler_resume(void);
void maintenance_scheduler_shutdown(void);
maintenance_scheduler_health maintenance_scheduler_health_copy(uint64_t tick);
void maintenance_scheduler_reset_for_tests(void);

#endif
