#ifndef DURIS_COLLECTOR_LISTING_PIPELINE_H
#define DURIS_COLLECTOR_LISTING_PIPELINE_H

#include "economy/collector_storage.h"

#include <cstddef>
#include <cstdint>

constexpr size_t COLLECTOR_LISTING_MAX_PENDING = 64;
constexpr size_t COLLECTOR_LISTING_MAX_COMPLETIONS = 64;

enum class collector_listing_outcome : uint8_t
{
	found,
	not_found,
	retryable_failure,
	invalid_data,
	cancelled,
};

enum class collector_listing_submit_outcome : uint8_t
{
	accepted,
	duplicate,
	invalid,
	capacity_exceeded,
	unavailable,
};

struct collector_listing_request
{
	uint64_t request_id = 0;
	uint64_t listing = 0;
};

struct collector_listing_result
{
	uint64_t request_id = 0;
	uint64_t listing = 0;
	collector_listing_outcome outcome = collector_listing_outcome::retryable_failure;
	unsigned int error_code = 0;
	collector_listing_detail detail;
};

struct collector_listing_pipeline_health
{
	uint64_t queued = 0;
	uint64_t inflight = 0;
	uint64_t completions = 0;
	uint64_t high_water = 0;
	uint64_t submitted = 0;
	uint64_t delivered = 0;
	uint64_t cancelled = 0;
	uint64_t found = 0;
	uint64_t not_found = 0;
	uint64_t retryable_failures = 0;
	uint64_t invalid_data = 0;
	bool running = false;
	bool stop_pending = false;
};

using collector_listing_execute_fn = collector_listing_result (*)(const collector_listing_request &,
								  void *);

bool collector_listing_pipeline_init(collector_listing_execute_fn execute = nullptr,
				     void *context = nullptr);
uint64_t collector_listing_pipeline_next_request_id(void);
collector_listing_submit_outcome
collector_listing_pipeline_submit(const collector_listing_request &request);
bool collector_listing_pipeline_cancel(uint64_t request_id);
size_t collector_listing_pipeline_pulse(collector_listing_result *results, size_t capacity);
void collector_listing_pipeline_shutdown(void);
collector_listing_pipeline_health collector_listing_pipeline_health_copy(void);
void collector_listing_pipeline_reset_for_tests(void);

#endif
